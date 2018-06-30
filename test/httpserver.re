/* Configuration */
let certfile = "cert.pem";
let privkey = "privkey.pem";
let port = 9876;
let password = "hello";

let log = s => Printf.printf("%s\n%!", s);

let startServer = (handler, sockaddr, nbconn) => {
  /* Set up socket */
  let listeningSocket = Unix.socket(Unix.PF_INET, Unix.SOCK_STREAM, 0);
  Unix.setsockopt(listeningSocket, Unix.SO_REUSEADDR, true);
  Unix.bind(listeningSocket, sockaddr);
  Unix.listen(listeningSocket, nbconn);

  let threadHandler = (socket) => {
    handler(socket);
    Ssl.shutdown(socket);
  };

  let ctx = Ssl.create_context(Ssl.SSLv23, Ssl.Server_context);
  Ssl.set_password_callback(ctx, _ => password);
  Ssl.use_certificate(ctx, certfile, privkey);

  log(Printf.sprintf("Listening on port %d...", port));
  while (true) {
    let (clientSocket, _clientAddr) = Unix.accept(listeningSocket);
    let sslSocket = Ssl.embed_socket(clientSocket, ctx);
    Ssl.accept(sslSocket);
    ignore(Thread.create(threadHandler, sslSocket));
  };
};

let handleConnection = socket => {
  let bufsize = 1024;
  let buf = Bytes.create(bufsize);
  log("New thread!");
  let loop = ref(true);
  while (loop^) {
    switch (Ssl.read(socket, buf, 0, bufsize)) {
    | exception Ssl.Read_error(_) => {
      log("A client has quit");
      Ssl.shutdown(socket);
      loop := false
    }
    | length => 
      let msg = Bytes.sub(buf, 0, length);
      log(Printf.sprintf(">> recieved '%S'", msg));
      Ssl.output_string(
        socket,
        "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Type: text/plain\r\n\r\nHello",
      );
    };
  };
};

let () = {
  Ssl_threads.init();
  Ssl.init();
  startServer(
    handleConnection,
    Unix.ADDR_INET(Unix.inet_addr_any, port),
    100,
  );
};