function foo(data) {
    document.body.innerHTML = "";
    document.body.innerHTML = data;
}

const socket = new WebSocket('ws://localhost:6060');

socket.onopen = function(e) {
    socket.send('ping');
}

socket.onmessage = function(e) {
    if(e.data === 'reload') {
	socket.close();
	location.reload();
    }
}

/*
fetch("./post", { method: "POST", body: "http://www.example.com/"} )
    .then(res => res.text())
    .then(data => foo(data));
*/
