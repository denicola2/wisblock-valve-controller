function Decoder(bytes, port) {
    var battery = (bytes[0]<<8 | bytes[1])/100;//Battery,units:V
    var interval_remain = (bytes[2]<<8 | bytes[3]);//Remaining seconds in the interval,units:Seconds
    var valve_state = (bytes[4] & 0x1);//Valve state

    return {
      BATTERY_V:battery,
      INTERVAL_REMAIN:interval_remain,
      VALVE_STATE:valve_state
    };
}