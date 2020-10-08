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
    text: "STONKS",
    fontFamily: "arial black",
    fontColor: "#695A42",
  },
  axisX: {
    interval: 1,
    intervalType: "year",
  },
  axisY: {
    valueFormatString: "$#",
    gridColor: "#B6B1A8",
    tickColor: "#B6B1A8",
  },
  toolTip: {
    shared: true
  },
  data: [
    {
      type: "stackedColumn",
      showInLegend: true,
      color: "#696661",
      name: "AMZN",
      dataPoints: amazon
    },
    {
      type: "stackedColumn",
      showInLegend: true,
      name: "FB",
      color: "#EDCA93",
      dataPoints: facebook
    },
    {
      type: "stackedColumn",
      showInLegend: true,
      name: "GOOGL",
      color: "#695A42",
      dataPoints: google
    },
    {
      type: "stackedColumn",
      showInLegend: true,
      name: "MSFT",
      color: "#B6B1A8",
      dataPoints: microsoft
    },
  ],
};

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
