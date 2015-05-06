// 参考:
// http://qiita.com/setzz/items/66b309a6c3c20d910218
// http://www.tettori.net/post/852/
// https://github.com/smallfield/iOS-Accelometer-Streamer/tree/master/server

var fs = require('fs');
var dgram = require('dgram');

var server = require('http').createServer(function(req, res) {
    res.writeHead(200, {'Content-Type': 'text/html'});
    var output = fs.readFileSync('./index.html', 'utf-8');
    res.end(output);
}).listen(8080);

var udpclient = dgram.createSocket('udp4');

var io = require('socket.io').listen(server);


io.sockets.on('connection', function(client) {

    client.on('message', function(message) {
        console.log(message);
    });

    client.on('motion', function(accel) {
        //console.log('got motion: x=' + accel.x + ', y=' + accel.y + ', z=' + accel.z);
        console.log('got motion: ' + accel.max);
        var pwm_duty = Math.floor(accel.max) + 20;
        if(pwm_duty < 0) { pwm_duty = 0; }
        if(pwm_duty > 254) { pwm_duty = 254; }
        var buf = new Buffer(1);
        buf[0] = pwm_duty;
        udpclient.send(buf, 0, buf.length, 12345, '127.0.0.1');
    });

    client.on('keepalive', function() {
        console.log('got keepalive');
        var buf = new Buffer(1);
        buf[0] = 0xff;
        udpclient.send(buf, 0, buf.length, 12345, '127.0.0.1');
    });

    client.on('disconnect', function() {
        console.log('disconnected');
    });
});

