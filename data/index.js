function addSeries(chart, seriesNum, date, data, redraw) {
  chart.data.datasets[seriesNum].data.push({x: date, y: parseFloat(data)});
  if (redraw)
    chart.update();
}

function processData(data, redraw=true) {
  let dataSpl = data.split(",");
  let date = new Date(parseInt(dataSpl[24])*1000);
  console.log(data,date)
  addSeries(dust, 0, date, dataSpl[9], false);
  addSeries(dust, 1, date, dataSpl[10], false);
  addSeries(dust, 2, date, dataSpl[11], false);
  addSeries(dust, 3, date, dataSpl[12], redraw);
  addSeries(dust2, 0, date, dataSpl[13], false);
  addSeries(dust2, 1, date, dataSpl[14], false);
  addSeries(dust2, 2, date, dataSpl[15], false);
  addSeries(dust2, 3, date, dataSpl[16], false);
  addSeries(dust2, 4, date, dataSpl[17], redraw);
  addSeries(dust3, 0, date, dataSpl[18], redraw);
  addSeries(gas, 0, date, dataSpl[2], false);
  addSeries(gas, 1, date, dataSpl[1], false);
  addSeries(gas, 2, date, dataSpl[3], redraw);
  addSeries(alt, 0, date, dataSpl[6], false);
  addSeries(alt, 1, date, dataSpl[21], false);
  addSeries(alt, 2, date, dataSpl[5], redraw);
  addSeries(temp, 0, date, dataSpl[4], false);
  addSeries(temp, 1, date, dataSpl[7], redraw);
  addSeries(hum, 0, date, dataSpl[8], redraw);
  
  dat.geometry.coordinates.push([parseInt(dataSpl[19])/1e7,parseInt(dataSpl[20])/1e7]);
  if (redraw) {
    map.getSource("gpsroute").setData(dat);
    let spd = parseFloat(dataSpl[23]);
    let hdg = parseFloat(dataSpl[22]);
    let canvas = document.getElementById("map-canvas");
    canvas.width = 120;
    canvas.height = 120;
    let ctx = canvas.getContext("2d");
    let startX = 0.5*canvas.clientWidth, startY = 0.5*canvas.clientHeight;
    let endX = spd*Math.sin(hdg/(2*Math.PI))*0.004*canvas.clientWidth+startX;
    let endY = spd*Math.cos(hdg/(2*Math.PI))*0.004*canvas.clientHeight+startY;
    ctx.beginPath();
    ctx.lineWidth=3;
    ctx.fillStyle="black";
    ctx.moveTo(endX, endY);
    ctx.lineTo(startX, startY);
    ctx.moveTo(endX+canvas.clientWidth*0.02, endY);
    ctx.arc(endX, endY, canvas.clientWidth*0.02, 0, 2*Math.PI, false);
    ctx.fill();
    ctx.stroke();
    ctx.setLineDash([5,5]);
    ctx.moveTo(startX+canvas.clientWidth*0.004*spd, startY);
    ctx.arc(startX,startY,canvas.clientWidth*0.004*spd,0,2*Math.PI,false);
    ctx.stroke();
    document.getElementById("speed").innerHTML = "V="+spd+"km/h";
  }
}

function setupWebsocket() {
  ws = new WebSocket("ws://"+window.location.hostname+":8080");

  ws.onmessage = function(event) {
    let data = JSON.parse(event.data);
    document.getElementById("RSSI").innerHTML = data["rssi"];
    document.getElementById("SNR").innerHTML = data["snr"];
    document.getElementById("freqErr").innerHTML = data["freqErr"];
    document.getElementById("data").innerHTML = data["data"];
    processData(data["data"]);
  };
  ws.onclose = function() {
    setTimeout(setupWebsocket, 1000);
  };
}

dust = new Chart(document.getElementById('dust'), {
  type: 'line',
  data: {
    datasets: [{
      label: 'PM 1',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'green',
      data: []
    },{
      label: 'PM 2.5',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'red',
      data: []
    },{
      label: 'PM 4',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'blue',
      data: []
    },{
      label: 'PM 10',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'orange',
      data: []
    }]
  },

  options: {
    title: {
      display: true,
      text: 'Staub'
    },
    legend: {
      position: 'right',
      align: 'start'
    },
    scales: {
      yAxes: [{
        scaleLabel: {
          display: true,
          labelString: "µg/m³"
        }
      }],
      xAxes: [{
        scaleLabel: {
          display: true,
          labelString: "Zeit"
        },
        type: 'time',
        time: {
          unit: 'second',
          distribution: 'linear'
        }
      }]
    }
  }
});

dust2 = new Chart(document.getElementById('dust2'), {
  type: 'line',
  data: {
    datasets: [{
      label: 'PM 0.5',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'yellow',
      data: []
    },{
      label: 'PM 1',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'green',
      data: []
    },{
      label: 'PM 2.5',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'red',
      data: []
    },{
      label: 'PM 4',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'blue',
      data: []
    },{
      label: 'PM 10',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'orange',
      data: []
    }]
  },

  options: {
    title: {
      display: true,
      text: 'Staub'
    },
    legend: {
      position: 'right',
      align: 'start'
    },
    scales: {
      yAxes: [{
        scaleLabel: {
          display: true,
          labelString: "1/m³"
        }
      }],
      xAxes: [{
        scaleLabel: {
          display: true,
          labelString: "Zeit"
        },
        type: 'time',
        time: {
          unit: 'second',
          distribution: 'linear'
        }
      }]
    }
  }
});

dust3 = new Chart(document.getElementById('dust3'), {
  type: 'line',
  data: {
    datasets: [{
      label: 'Durchschnittliche Staubgröße',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'green',
      data: []
    }]
  },

  options: {
    title: {
      display: true,
      text: 'Durchschnittliche Staubgröße'
    },
    legend: {
      display: false
    },
    scales: {
      yAxes: [{
        scaleLabel: {
          display: true,
          labelString: "µg"
        }
      }],
      xAxes: [{
        scaleLabel: {
          display: true,
          labelString: "Zeit"
        },
        type: 'time',
        time: {
          unit: 'second',
          distribution: 'linear'
        }
      }]
    }
  }
});

gas = new Chart(document.getElementById('gas'), {
  type: 'line',
  data: {
    datasets: [{
      label: 'CO',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'yellow',
      data: []
    },{
      label: 'CH4',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'green',
      data: []
    },{
      label: 'O3',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'red',
      data: []
    }]
  },

  options: {
    title: {
      display: true,
      text: 'Gaskonzentrationen'
    },
    legend: {
      position: 'right',
      align: 'start'
    },
    scales: {
      yAxes: [{
        scaleLabel: {
          display: true,
          labelString: "Sensorenspannung"
        }
      }],
      xAxes: [{
        scaleLabel: {
          display: true,
          labelString: "Zeit"
        },
        type: 'time',
        time: {
          unit: 'second',
          distribution: 'linear'
        }
      }]
    }
  }
});

alt = new Chart(document.getElementById('alt'), {
  type: 'line',
  data: {
    datasets: [{
      label: 'Höhe Druck',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'yellow',
      yAxisID: "m",
      data: []
    },{
      label: 'Höhe GPS',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'green',
      yAxisID: "m",
      data: []
    },{
      label: 'Druck',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'blue',
      yAxisID: "Pa",
      data: []
    }]
  },

  options: {
    title: {
      display: true,
      text: 'Höhe und Druck'
    },
    legend: {
      position: 'right',
      align: 'start'
    },
    scales: {
      yAxes: [{
        id: 'm',
        scaleLabel: {
          display: true,
          labelString: "m"
        }
      },{
        id: "Pa",
        position: "right",
        scaleLabel: {
          display: true,
          labelString: "Pa"
        }
      }],
      xAxes: [{
        scaleLabel: {
          display: true,
          labelString: "Zeit"
        },
        type: 'time',
        time: {
          unit: 'second',
          distribution: 'linear'
        }
      }]
    }
  }
});

temp = new Chart(document.getElementById('temp'), {
  type: 'line',
  data: {
    datasets: [{
      label: 'Intern BMP',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'green',
      data: []
    },{
      label: 'Extern',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'blue',
      data: []
    }]
  },

  options: {
    title: {
      display: true,
      text: 'Temperatur'
    },
    legend: {
      position: 'right',
      align: 'start'
    },
    scales: {
      yAxes: [{
        scaleLabel: {
          display: true,
          labelString: "°C"
        }
      }],
      xAxes: [{
        scaleLabel: {
          display: true,
          labelString: "Zeit"
        },
        type: 'time',
        time: {
          unit: 'second',
          distribution: 'linear'
        }
      }]
    }
  }
});

hum = new Chart(document.getElementById('hum'), {
  type: 'line',
  data: {
    datasets: [{
      label: 'Luftfeuchtigkeit',
      backgroundColor: 'rgba(0,0,0,0)',
      borderColor: 'green',
      data: []
    }]
  },

  options: {
    title: {
      display: true,
      text: 'Luftfeuchtigkeit'
    },
    legend: {
      display: false
    },
    scales: {
      yAxes: [{
        scaleLabel: {
          display: true,
          labelString: "%"
        }
      }],
      xAxes: [{
        scaleLabel: {
          display: true,
          labelString: "Zeit"
        },
        type: 'time',
        time: {
          unit: 'second',
          distribution: 'linear'
        }
      }]
    }
  }
});

map = new mapboxgl.Map({
  container: 'map',
  center: [8.106,51.571],
  zoom: 7,
  minZoom: 7,
  maxZoom: 10,
  style: style
});

dat = {
  "type": "Feature",
  "properties": {},
  "geometry": {
    "type": "LineString",
    "coordinates": []
  }
};

map.on('load', function () {
  map.addSource("gpsroute", {
    "type": "geojson",
    "data": dat
  });

  map.addLayer({
    "id": "route",
    "type": "line",
    "source": "gpsroute",
    "layout": {
      "line-join": "round",
      "line-cap": "round"
    },
    "paint": {
      "line-color": "rgb(247, 137, 43)",
      "line-width": 6
    }
  });

  xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      let linesSpl = this.responseText.split(/\r?\n/).filter(v => v.length != 0);;
      for (let line of linesSpl.slice(0, -1))
        processData(line, false);
      processData(linesSpl[linesSpl.length-1]);
    }
  };
  xhttp.open("GET", "getData", true);
  xhttp.send();

  setupWebsocket();
});

map.on('mousemove', function (e) {
  document.getElementById('info').innerHTML = e.lngLat.lat.toFixed(4) + "°, " + e.lngLat.lng.toFixed(4) + "°";
});

map.addControl(new mapboxgl.NavigationControl());
