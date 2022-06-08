// Sample Datacake encoder to initiate a watering interval.
// - Add a custom "INTERVAL_CONFIGURE" (unit:seconds) measurement to the device fields in Datacake.
// - Trigger a downlink and provide the desired interval.

function Encoder(measurements, port) {
    var interval = measurements["INTERVAL_CONFIGURE"].value;
    
    var str = 'AT+VLVI=' + interval;
    var encoded = str.split ('').map (function (c) { return c.charCodeAt (0); })
    
    return encoded;
}