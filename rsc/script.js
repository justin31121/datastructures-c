function foo(data) {
    document.body.innerHTML = "";
    document.body.innerHTML = data;
}

const socket = new WebSocket('ws://localhost:6060');

socket.onopen = function(e) {
    socket.send('foo');
}

socket.onmessage = function(e) {
    console.log(e.data, e.data.length);
}

/*
fetch("./post", { method: "POST", body: "http://www.example.com/"} )
    .then(res => res.text())
    .then(data => foo(data));
*/
