#!/usr/bin/env node

var spawn = require('child_process').spawn;
var net = require('net');
var util = require('util');

var Harness = function () {
    this._output = '';
    this._errors = '';
};

var command = 'qemu-system-arm';
var args = [
    '-M', 'versatilepb',
    '-cpu', 'arm1176',
    '-nographic',
    '-kernel', 'target/kernel',
    '-serial', 'mon:stdio',
    '-serial', 'tcp::4444,server'
];

Harness.prototype.run = function () {
    var qemu = this._process = spawn(command, args);

    qemu.stdout.setEncoding('utf8');
    qemu.stderr.setEncoding('utf8');

    qemu.stdout.on('data', this.handleStdoutData.bind(this));
    qemu.stderr.on('data', this.handleStderrData.bind(this));

    this._process.on('exit', function (code, signal) {
        if (code == null) {
            console.log(
                'process exited abnormally with ' + signal + ', to debug (until node is fixed) run:\n' +
                command + ' ' + args.join(' ')
            );
        }
    });
};

Harness.prototype.stop = function () {
    console.log('stopping...');

    this._process.stdin.write('\x01x');

    var killTimeout = setTimeout(function () {
        console.error('timed out, sending SIGKILL');
        this._process.kill('SIGKILL');
    }.bind(this), 8000);

    this._process.on('exit', function (status) {
        clearTimeout(killTimeout);
    });
};

Harness.prototype.handleStdoutData = function (data) {
    this._output += data;
    while (/\n/.test(this._output)) {
        this._output = this._output.replace(/^([^\n]+)\n/, function (match, line) {
            if (/QEMU waiting for connection on: tcp:0.0.0.0:\d+,server/.test(line)) {
                this.connectSerial1();
            }
            else if (/^debug: /.test(line)) {
                console.log(line);
            }
            else if (/QEMU: Terminated/.test(line)) {
                console.log(line);
            }
            else {
                console.log('throwing');
                throw new Error('unexpected output from QEMU: ' + line);
            }
            return '';
        }.bind(this));
    }
};

Harness.prototype.handleStderrData = function (data) {
    this._errors += data;
    this._errors.replace(/^([^\n]+)\n/g, function (match, line) {
        console.error(line);
    });
};

Harness.prototype.connectSerial1 = function () {
    var client = net.connect(4444, function () {
        client.setEncoding('utf8');
        client.on('data', this.handleSerial1Data.bind(this));
    }.bind(this));
};

Harness.prototype.handleSerial1Data = function (data) {
    console.log('got data (UART1): ' + data);
};

var harness = new Harness();

harness.run();

process.on('SIGINT', function () {
    console.log('');
    harness.stop();
});

process.on('uncaughtException', function (e) {
    console.error(e);
    harness.stop();
});


