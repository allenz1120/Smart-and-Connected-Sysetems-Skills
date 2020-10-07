const SerialPort = require("serialport");
const port = new SerialPort("COM3", {
  baudRate: 115200,
});

port.on("readable", function () {
  console.log("Data: %s", port.read());
});
