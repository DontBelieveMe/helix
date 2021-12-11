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

  for (let result of globalReport.Tests) {
    labels.push(result.Compilation.SourceFile);
    times.push(result.Compilation.CompilationTime);
    stati.push(result.Status);
  }

    // Graphs
    var ctx = document.getElementById('overviewChart')
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
            return value === 0 ? "green" : value === 1 ? "red" : "purple";
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
  
    for (let result of globalRegressions) {
      labels.push(result.When);
      passes.push(result.Passes);
      fails.push(result.Fails);
      skipped.push(result.Skipped ? result.Skipped : 0);   
    }

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