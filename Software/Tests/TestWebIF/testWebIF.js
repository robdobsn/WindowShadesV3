var http = require("http");
var Express = require("express");
var path = require("path");
var request = require('request');
var moment = require('moment');
var bodyParser = require('body-parser');

var app = Express();
app.set("port", process.env.PORT || 24782);
app.set("views", path.join(__dirname, "views"));
app.set("view engine", "jade");

app.use(function(req, res, next) {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  return next();
});

var shadesCfg = { numShades: 2, name: "Office Blinds", shades: [
    { name: "Shade1", num: "1" },
    { name: "Shade2", num: "2" },
    { name: "Shade3", num: "3" },
    { name: "Shade4", num: "4" },
    { name: "Shade5", num: "5" },
    { name: "Shade6", num: "6" }
  ] };

app.use(bodyParser.urlencoded({
  extended: false
}));

var jsonParser = bodyParser.json();

app.get("/q/q", function(req, res) {
    console.log("Got Q");
    res.send(shadesCfg);
});

app.get("/blind/:idx?/:cmd?/*", function(req, res) {
    var idx = req.params.idx !== void 0 ? req.params.idx : "";
    var cmd = req.params.cmd !== void 0 ? req.params.cmd : "";
    console.log("Got Blind " + idx + " " + cmd + " ");
    res.send({ rslt: "ok" });
});

app.get("/shadecfg/:windowName?/:numShades?/:name1?/:name2?/:name3?/:name4?/:name5?/:name6?", function(req, res) {
    var windowName = req.params.windowName !== void 0 ? req.params.windowName : "";
    var numShades = req.params.numShades !== void 0 ? req.params.numShades : "";
    var name1 = req.params.name1 !== void 0 ? req.params.name1 : "";
    var name2 = req.params.name2 !== void 0 ? req.params.name2 : "";
    var name3 = req.params.name3 !== void 0 ? req.params.name3 : "";
    var name4 = req.params.name4 !== void 0 ? req.params.name4 : "";
    var name5 = req.params.name5 !== void 0 ? req.params.name5 : "";
    var name6 = req.params.name6 !== void 0 ? req.params.name6 : "";
    console.log("ShadeCfg " + windowName + " " + numShades + " " + name1 + " " + name2 + " " + name3 + " " + name4 + " " + name5 + " " + name6 + " ");
    shadesCfg.name = windowName;
    shadesCfg.numShades = numShades;
    shadesCfg.shades[0].name = name1;
    shadesCfg.shades[1].name = name2;
    shadesCfg.shades[2].name = name3;
    shadesCfg.shades[3].name = name4;
    shadesCfg.shades[4].name = name5;
    shadesCfg.shades[5].name = name6;
    res.send(shadesCfg);
});

app.use(Express.static('public'));

app.use(function(req, res) {
  res.render("404", {
    url: req.url
  });
});

http.createServer(app).listen(app.get("port"), function() {
  console.log(moment().format() + " " + "TestWeb server listening on port " + app.get("port"));
});
