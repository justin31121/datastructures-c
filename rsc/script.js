function foo(data) {
    document.body.innerHTML = "";
    document.body.innerHTML = data;
}

fetch("./post", { method: "POST", body: "http://www.example.com/"} )
    .then(res => res.text())
    .then(data => foo(data));
