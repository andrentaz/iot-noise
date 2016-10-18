/**
 * Express Server version - simple post handler
 */

// Packages importation
var express = require('express');
var bodyParser = require('body-parser');
var fs = require('fs');

// Server setup
var app = express();
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

// callback for post to upload
function callback_csv (req, res) {
    var id = req.body.id;
    var vreff = req.body.vreff;
    var tmp = req.body.tmp;
    var spl = req.body.spl;

    // create the line
    var content = [id, vreff, tmp, spl].toString() + "\n";
    var filepath = 'csv/upload/' + id + ".csv";

    // write the file
    fs.appendFile(filepath, content, function (err) {
        if (err) {
            // send error response to client
            res.send (JSON.stringify({
                file: req.body.id,
                status: err
            }));
            
            // return
            return console.log(err);
        }
        console.log("File '" + id + ".csv' successfully saved!");
            
        // send response to the client
        res.send (JSON.stringify({
            file: req.body.id,
            status: "OK"
        }));
    });
}

// initially, for debug purposes 
function callback_get (req, res) {
    console.log("Got a request");
    res.send("Received\n");
}

// routing
app.post('/csv/upload', callback_csv);
app.get('/', callback_get);

// initialize Server
app.listen(8080, function () {
    console.log('Server started at http://localhost:8080');
});