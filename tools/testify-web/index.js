(function () {
  'use strict'
  
  var horizonalLinePlugin = {
    afterDraw: function(chartInstance) {
      var yScale = chartInstance.scales["y-axis-0"];
      var canvas = chartInstance.chart;
      var ctx = canvas.ctx;
      var index;
      var line;
      var style;
  
      if (chartInstance.options.horizontalLine) {
        for (index = 0; index < chartInstance.options.horizontalLine.length; index++) {
          line = chartInstance.options.horizontalLine[index];
 
          if (!line.style) {
            style = "rgba(169,169,169, .6)";
          } else {
            style = line.style;
          }
          let yValue = 0;
          if (line.y) {
            yValue = yScale.getPixelForValue(line.y);
          } else {
            yValue = 0;
          }
  
          ctx.lineWidth = 1;
  
          if (yValue) {
            ctx.beginPath();
            ctx.moveTo(0, yValue);
            ctx.lineTo(canvas.width, yValue);
            ctx.strokeStyle = style;
            ctx.stroke();
          }
  
          if (line.text) {
            ctx.fillStyle = "black";
            ctx.fillText(line.text, 0, yValue - 10);
          }
        }
        return;
      };
    }
  };
  Chart.pluginService.register(horizonalLinePlugin);

  feather.replace({ 'aria-hidden': 'true' });

  let labels = [];
  let times = [];
  let stati = [];

  let npasses=0;
  let nfails=0;
  let nskipped=0;

  for (let result of results) {
    var myTbody = document.querySelector(".results-table>tbody");
    var newRow = myTbody.insertRow();

    let filename = document.createElement("a");
    filename.innerText = result.TestFile;
    filename.href = "vscode://file/C:/helix/" + result.TestFile;
    newRow.insertCell().appendChild(filename);

    let statusCell = newRow.insertCell();
    statusCell.append(result.Status);
    if (result.Status === "pass") {
      statusCell.style.color = "green";
      npasses+=1;
    }
    else if (result.Status === "fail") {
      statusCell.style.color = "red";
      nfails+=1;
    } else if (result.Status === "skipped") {
      statusCell.style.color = "purple";
      nskipped+=1;
    }

    newRow.insertCell().append(result.ExecutionTime);
    newRow.insertCell().append(result.Tags);
    labels.push(result.TestFile);
    times.push(result.ExecutionTime);
    stati.push(result.Status);
  }

    // Graphs
    var ctx = document.getElementById('myChart')
    // eslint-disable-next-line no-unused-vars
    var myChart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: labels,
        datasets: [{
          data: times,
          lineTension: 0,
          backgroundColor: 'transparent',
          borderColor: '#007bff',
          borderWidth: 1,
          pointBackgroundColor: function(context) {
            var index = context.dataIndex;
            var value = stati[index];
            return value === "pass" ? "green" : value === "fail" ? "red" : "purple";
          },
          label: "Compile Time"
        }]
      },
      options: {
        scales: {
          yAxes: [{
            ticks: {
              beginAtZero: true
            }
          }]
        },
        legend: {
          display: true
        },
        horizontalLine: [{
          y: 2000,
          style: "rgba(175, 0, 0, .4)",
          text: "Timeout"
        }]
      }
    })

    labels = [];
    let passes = [];
    let fails = [];
    let skipped = [];
  
    for (let result of testsummaries) {
      labels.push(result.When);
      passes.push(result.Passes);
      fails.push(result.Fails);
      skipped.push(result.Skipped ? result.Skipped : 0);   
    }

    gtotal.innerText = npasses+nfails+nskipped;
    gpasses.innerText = npasses;
    gfails.innerText = nfails;
    gskipped.innerText = nskipped;

    ctx = document.getElementById('regressionChart')
    myChart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: labels,
        datasets: [
          {
            data: passes,
            lineTension: 0,
            backgroundColor: 'green',
            borderColor: '#007bff',
            borderWidth: 1,
            label: "Passes",
            fill: true
          },
 
          {
            data: fails,
            lineTension: 0,
            backgroundColor: 'red',
            borderColor: '#007bff',
            borderWidth: 1,
            label: "Fails",
            fill: true
          },
         {
            data: skipped,
            lineTension: 0,
            backgroundColor: 'purple',
            borderColor: '#007bff',
            borderWidth: 1,
            label: "Skipped",
            fill: true,
          },

        ]
      },
      options: {
        responsive: true,
        legend: {
          display: true
        },
        scales: {
          xAxes: [{
            stacked: true
          }],
          yAxes: [{
            stacked: true
          }]
        }
      }
    })
  
})()