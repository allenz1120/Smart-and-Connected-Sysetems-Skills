//Allen Zou
//10/7/2020

var app = require('express')();
var http = require("http").Server(app);
var fs = require("fs");
const path = require("path");
//io is the websocket object
const io = require("socket.io")(http);

var amazon = [];
var facebook = [];
var google = [];
var microsoft = [];

var csv_lines = fs.readFileSync((__dirname + '/stocks.csv'), 'utf8').toString().split("\n");
var data = [];
for (i = 1; i < csv_lines.length; i++) {
	data = csv_lines[i].split(",");
	console.log("csv row is ---", data);
	switch (data[1]) {
		case 'AMZN':
			amazon.push({
				x: Number(data[0]),
				y: Number(data[2])
			});
			break;
		case 'FB':
			facebook.push({
				x: Number(data[0]),
				y: Number(data[2])
			});
			break;
		case 'GOOGL':
			google.push({
				x: Number(data[0]),
				y: Number(data[2])
			});
			break;
		case 'MSFT':
			microsoft.push({
				x: Number(data[0]),
				y: Number(data[2])
			});
			break;
		default:
			console.log("Error retrieving data");
			break;
	}
}

var chartSettings = {
	animationEnabled: true,
	title: {
		text: "Stonky stonks"
	},
	axisX: {
		interval: 1,
		intervalType: "year"
	},
	axisY: {
		title: "Closing Price",
		prefix: "$"
	},
	legend: {
		cursor: "pointer",
		fontSize: 16,
		itemclick: toggleDataSeries
	},
	toolTip: {
		shared: true
	},
	data: [{
		name: "Amazon",
		type: "spline",
		yValueFormatString: "$#",
		showInLegend: true,
		dataPoints: amazon
	},
	{
		name: "Facebook",
		type: "spline",
		yValueFormatString: "$#",
		showInLegend: true,
		dataPoints: facebook
	},
	{
		name: "Google",
		type: "spline",
		yValueFormatString: "$#",
		showInLegend: true,
		dataPoints: google
	},
	{
		name: "Microsoft",
		type: "spline",
		yValueFormatString: "$#",
		showInLegend: true,
		dataPoints: microsoft
	}]
};

function toggleDataSeries(e) {
	if (typeof (e.dataSeries.visible) === "undefined" || e.dataSeries.visible) {
		e.dataSeries.visible = false;
	}
	else {
		e.dataSeries.visible = true;
	}
	chart.render();
}

app.get('/', function (req, res) {
	res.sendFile(__dirname + '/canvasJS.html');
	app.get('/data', function (req, res) {
		res.sendFile(__dirname + '/stocks.csv');
	});
});

io.on("connect", (socket) => {
	console.log("CONNECTED");
	let counter = 0;
	io.emit("", chartSettings);

});

http.listen(8080, () => {
	console.log("go to http://localhost:8080");
});
