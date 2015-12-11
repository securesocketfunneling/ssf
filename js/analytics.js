(function($, ga) {
    $(document).ready(function() {
        $('a[data-type=download]').on('click', function(event) {
            var element = $(this);
            ga('send', 'event', 'Download', 'download', element.data('file'));
        });
    });
})(jQuery, ga);