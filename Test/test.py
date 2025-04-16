import requests

def send_chunked():
    def chunked_data():
        chunks = ["First chunk", "Second chunk", "Third chunk"]
        for chunk in chunks:
            yield f"{len(chunk):X}\r\n{chunk}\r\n"
        yield "0\r\n\r\n"
    
    url = "http://localhost:8080/test-upload"
    headers = {"Transfer-Encoding": "chunked"}
    response = requests.post(url, headers=headers, data=chunked_data())
    print(response.text)

send_chunked()