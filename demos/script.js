console.log("Hello, JavaScript!");

const data = {
    'foo': 69
}

fetch("./post", {
    method: "POST",
    body: JSON.stringify(data)
}).then(res => res.json())
    .then(data => console.log(data));
