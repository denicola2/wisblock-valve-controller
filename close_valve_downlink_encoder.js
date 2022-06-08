// Sample Datacake encoder to force valve state to closed.

function Encoder(measurements, port) {
    var str = 'AT+VLVS=0';
    var encoded = str.split ('').map (function (c) { return c.charCodeAt (0); })
    
    return encoded;
}