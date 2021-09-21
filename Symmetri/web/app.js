var ws;

function add(text) {
    var chat = $('#chat-area');
    chat.text(text);
}

$(function begin() {
    $('#form').submit(function onSubmit(e) {
        var ci = $("");
        ws.send(ci);
        ci.val("");
        e.preventDefault();
    });
    // Test the protocol support...
    ws = new WebSocket('ws://' + document.location.host + '/start');
    ws.onopen = function () {
        console.log('onopen');
    };
    ws.onclose = function () {
        add('Lost connection.');
        console.log('onclose');
    };
    ws.onmessage = function (message) {
       
        add(message.data);
    };
    ws.onerror = function (error) {
        add("ERROR: " + error);
    };
        
    const spec = "http://localhost:2222/load_scheme.json";
    vegaEmbed('#vis', spec, {defaultStyle: true})
    .then(function(result) {
        const view = result.view;
    
        conn = new WebSocket('ws://' + document.location.host + '/transition_data');
        var window = 15 // sec
        // insert data as it arrives from the socket
        conn.onmessage = function(message) {
        // var fooField = vega.field('end');
        data = vega.read(message.data, {type: 'csv', 'header': ['thread','start','end','task'] ,parse: {'thread': 'number', 'start': 'number', 'end':'number', 'task':'string'}});
        var changeSet = vega
            .changeset()
            .insert(data)
            .remove(function (t) {
                return t.end < (vega.peek(view.data('table')).end - window*1000000000)
            });
            view.change('table', changeSet).run();
        }
        }).catch(console.warn);
});
