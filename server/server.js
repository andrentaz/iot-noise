/**
 * Express Server version - simple post handler
 */

// Packages importation
var express = require('express');
var bodyParser = require('body-parser');
var fs = require('fs');
var moment = require('moment');

// Server setup
var app = express();
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

// callback for post to upload
function cb_postcsv (req, res) {
    // get the timestamp
    var timestamp = moment();

    // get the attibutes 
    var id = req.body.id;
    var vref = req.body.vref;
    var dbref = req.body.dbref;
    var tmvlist = req.body.tmvlist;

    // log it
    console.log('POST Request for file: ' + id + '.csv');
    console.log('Request for saving: ' + tmvlist.length/2 + ' records');
    console.log('Received time: ' + timestamp.format());

    // create the line
    var content = '';
    var filepath = 'csv/upload/' + id + ".csv";

    var stime = timestamp;
    for (var i = tmvlist.length-1; i > 0; i--) {
        // get the variables
        var valdb = tmvlist[i];
        var msec = tmvlist[i-1];
        stime = stime.subtract(msec, 'ms');
        var ftt = stime.format('YYYY/MM/dd HH:mm:ss.SSSS');

        // print the line in content
        content = [id,vref,dbref,ftt,msec,valdb].toString() + '\n' + content;
        i--;
    }

    // write the file
    fs.appendFile(filepath, content, function (err) {
        if (err) {
            // send error response to client
            res.send (JSON.stringify(err));
            
            // return
            return console.log(err);
        }
        console.log("File '" + id + ".csv' successfully saved!");
            
        // send response to the client
        res.send (JSON.stringify({
            file: req.body.id,
            time: 'Received at: ' + timestamp.format().split('T').join(' '),
            status: "OK"
        }));
    });
}

// Send file callback
function cb_getcsv(req, res) {
    // get the file Id to be sent
    var id = req.query.fileId;
    console.log("GET Request for file: " + id + ".csv");
    var filepath = './csv/upload/' + id + '.csv';
    res.sendFile(filepath, { root: __dirname });
    console.log("File sent");
}

// routing
app.post('/csv/upload', cb_postcsv);
app.get('/csv/upload', cb_getcsv);

// initialize Server
app.listen(8080, function () {
    console.log('Server started at http://localhost:8080');
});