var myAPIKey = '<YOUR_API_KEY>'; // Replace with your NextBillion.ai API key

function sendToPebble(distance, duration, timestamp) {
  var payload = {
    'TRAVEL_ICON_KEY': 0,
    'TRAVEL_DISTANCE_KEY': distance,
    'TRAVEL_DURATION_KEY': duration
  };
  if (timestamp) {
    payload['TRAVEL_TIMESTAMP_KEY'] = timestamp;
  }

  Pebble.sendAppMessage(payload, function(e) {
    console.log('Successfully sent to Pebble');
  }, function(e) {
    console.log('Failed to send to Pebble: ' + JSON.stringify(e));
  });
}

function fetchTravelData() {
  if (!myAPIKey || myAPIKey === '' || myAPIKey === 'REPLACE_ME') {
    sendToPebble('No Key', 'Check JS', '');
    return;
  }

  sendToPebble('...', '...', '');

  var origin = '40.1177,-83.1265';
  var destination = '39.9980,-82.8919'; // CMH Airport
  
  // Using the provided flexible navigation endpoint
  var url = 'https://api.nextbillion.io/navigation?option=flexible&key=' + myAPIKey + '&origin=' + origin + '&destination=' + destination;

  console.log('Fetching: ' + url);

  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.timeout = 10000; // 10 second timeout
  
  req.onload = function () {
    if (req.readyState === 4) {
      if (req.status === 200) {
        console.log('Response: ' + req.responseText);
        var response = JSON.parse(req.responseText);
        // Flexible API returns 'status' and 'routes'
        if (response.status === 'Ok' && response.routes && response.routes.length > 0) {
          var route = response.routes[0];
          
          // Distance in miles (meters to miles)
          var distanceMiles = (route.distance / 1609.34).toFixed(1);
          
          // Duration in HH:MM
          var durationSeconds = route.duration;
          var hours = Math.floor(durationSeconds / 3600);
          var minutes = Math.floor((durationSeconds % 3600) / 60);
          var durationFormatted = (hours < 10 ? '0' : '') + hours + ':' + (minutes < 10 ? '0' : '') + minutes;

          // Request date/time
          var now = new Date();
          var timeStr = (now.getHours() < 10 ? '0' : '') + now.getHours() + ':' + (now.getMinutes() < 10 ? '0' : '') + now.getMinutes();
          var dateStr = (now.getMonth() + 1) + '/' + now.getDate();
          var timestamp = dateStr + ' ' + timeStr;

          sendToPebble(distanceMiles + ' mi', durationFormatted, '@ ' + timestamp);
        } else {
          sendToPebble('Error', 'No routes', '');
        }
      } else {
        console.log('Status: ' + req.status);
        sendToPebble('HTTP ' + req.status, 'Error', '');
      }
    }
  };
  
  req.onerror = function (e) {
    console.log('Request Error');
    sendToPebble('Net Error', 'Check Connection');
  };

  req.ontimeout = function () {
    console.log('Request Timeout');
    sendToPebble('Timeout', 'API took too long');
  };

  req.send(null);
}

Pebble.addEventListener('ready', function (e) {
  console.log('connect!' + e.ready);
  fetchTravelData();
  
  // Fetch every 60 seconds
  setInterval(function() {
    fetchTravelData();
  }, 60000);
});

Pebble.addEventListener('appmessage', function (e) {
  fetchTravelData();
  console.log('message!');
});

Pebble.addEventListener('webviewclosed', function (e) {
  console.log('webview closed');
  console.log(e.type);
  console.log(e.response);
});
