(function($) {
    "use strict";
    $(document).ready(function() {
        var showTab = function(name) {
            $("#menu a[href='#"+name+"']").tab('show');
        }
        var tabSelection = function(name) {
            $("[data-action='select-"+name+"']").click(function(e) {
              e.preventDefault();
              e.stopPropagation();
              showTab(name);
            });
        }
        
        // All elements with attribute data-action='select-...' will display associated tab
        $("#menu a").each(function(index, tabLink) {
          tabLink = $(tabLink);
          if (tabLink.attr('href').length > 1) {
            tabSelection(tabLink.attr('href').substr(1));
          }
        });
        
        // Save current page in hash for reload or open tab behaviour
        $("ul.nav-tabs > li a").on("shown.bs.tab", function(e) {
            e.preventDefault();
            e.stopPropagation();
            var hash = $(e.target).attr("href").substr(1);
            window.location.hash = hash;
            $(window).scrollTop(0);
            
            if (hash == "performances") {
              $('#performances-graph').highcharts({
                  credits: {
                      enabled: false
                  },
                  colors: [ '#244363', '#93C54B'],
                  chart: {
                      type: 'column'
                  },
                  title: {
                      text: ''
                  },
                  xAxis: {
                      categories: [ 'Cipher' ],
                      title: { text: '' }
                  },
                  yAxis: {
                      type: 'category',
                      endOnTick: false,
                      tickInterval: 25,
                      max: 1000,
                      title: { text: 'Mbits/s' }
                  },
                  tooltip: {
                      pointFormat: '{series.name}: <strong>{point.y}</strong> Mbits/s'
                  },
                  series: [
                      {
                          name: 'SSH',
                          data: [ 125 ]
                      },
                      {
                          name: 'SSF',
                          data: [ 600 ]
                      }/*,
                      {
                          name: 'SSF - No cipher',
                          data: [ 1000 ]
                      }*/
                  ]
              });
            }
        });
        
        $("#back-top").click(function(e) {
            e.preventDefault();
            e.stopPropagation();
            $(window).scrollTop(0);
        });   
        
        // After load, select tab with provided hash
        if (window.location.hash) {
            var hash = window.location.hash.substr(1);
            var tabMenu = [];
            if (hash && $("#menu a[href='#"+hash+"']").length) {
                showTab(hash);
            } else {
                showTab("home");
            }
        } else {
            showTab("home");
        }
        
        // Change tab on hash change
        $(window).on('hashchange', function(e) {
          var hash = window.location.hash.substr(1);
          if (hash && $("#menu a[href='#"+hash+"']").length) {
              showTab(hash);
          } else {
              showTab("home");
          }
        });
    });
})(jQuery);