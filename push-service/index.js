const express = require("express");
const mongoose = require("mongoose");
const bodyparser = require("body-parser");

const app = express();

app.use(bodyparser.urlencoded({ extended: true }));
app.use(express.static("public"));

mongoose.Promise = global.Promise;
mongoose.connect("mongodb://localhost/push-service", { useNewUrlParser: true })
		.then(() => console.log("connected mongoose"))
		.catch(e => console.log(e));
const db = mongoose.connection;

var UserScheme = mongoose.Schema({
	userhash: String,
	token: String
});
var DeviceScheme = mongoose.Schema({
	device: String,
	userhash: String
});

var User = mongoose.model("User", UserScheme);
var Device = mongoose.model("Device", DeviceScheme);

const admin = require("firebase-admin");
const serviceAccount = require("./key/fcm-admin-key.json");

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL: "https://sdoor-2e293.firebaseio.com"
});

function add_user(user, token) {
	User.findOne({ user: user }).exec((err, result) => {
		if(!result) {
			User.insert({ userhash: user, token: token });
		}
	});
}

function add_device(user, device) {
	Device.findOne({ device: device }).exec((err, result) => {
		if(!result) {
			Device.insert({ device: device, userhash: user });
		}
	});
}

function push(token, body) {
	// This registration token comes from the client FCM SDKs.
	var message = {
	  data: {
	    title: "alert",
	    body: body
	  },
	  token: token
	};

	// Send a message to the device corresponding to the provided
	// registration token.
	admin.messaging().send(message)
	  .then((response) => {
	    // Response is a message ID string.
	    console.log('Successfully sent message:', response);
	  })
	  .catch((error) => {
	    console.log('Error sending message:', error);
	  });
}

app.post("/add_user", (req, res) => {
	var userhash = req.body.user;
	var token = req.body.token;

	if(typeof userhash !== "string") {
		res.send("no hack");
		return;
	}

	add_user(userhash, token);
	res.send("user added");
});

app.post("/add_device", (req, res) => {
	var userhash = req.body.user;
	var device = req.body.device;

	if(typeof device !== "string") {
		res.send("no hack");
		return;
	}

	add_device(userhash, device);
	res.send("device added");
});

app.post("/alert", (req, res) => {
	var device = req.body.device;
	var msg = req.body.msg;
	if(typeof device !== "string") {
		res.send("no hack");
		return;
	}

	Device.findOne({ device: device }).exec((err, result) => {
		if(result){
			User.findOne({ userhash: result.userhash }).exec((e, r) => {
				push(r.token, msg);
			});
		}
	});

	res.send("push");
});

app.get("/", (req, res) => {
	res.send("push-service");
})

app.listen(2222);
