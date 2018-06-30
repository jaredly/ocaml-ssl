

/* Configuration */
let certfile = "cert.pem";
let privkey = "privkey.pem";
let port = 9876;
let password = "hello";

let log = s => Printf.printf("%s\n%!", s);

let establish_threaded_server = (server_handler, sockaddr, nbconn) => {
  log("establishing server");
  let domain =
    switch (sockaddr) {
    | Unix.ADDR_UNIX(_) => Unix.PF_UNIX
    | Unix.ADDR_INET(_, _) => Unix.PF_INET
    };

  let listeningSocket = Unix.socket(domain, Unix.SOCK_STREAM, 0);
  let handle_connexion = ((socket, caller)) => {
    /* let inet_addr_of_sockaddr =
      fun
      | Unix.ADDR_INET(n, _) => n
      | Unix.ADDR_UNIX(_) => Unix.inet_addr_any;

    let inet_addr = inet_addr_of_sockaddr(caller);
    let ip = Unix.string_of_inet_addr(inet_addr);
    log(Printf.sprintf("openning connection for [%s]", ip)); */
    server_handler(socket);
    Ssl.shutdown(socket);
  };

  let ctx = Ssl.create_context(Ssl.SSLv23, Ssl.Server_context);
  if (password != "") {
    Ssl.set_password_callback(ctx, _ => password);
  };
  Ssl.use_certificate(ctx, certfile, privkey);
  Unix.setsockopt(listeningSocket, Unix.SO_REUSEADDR, true);
  Unix.bind(listeningSocket, sockaddr);
  Unix.listen(listeningSocket, nbconn);
  /* let ssl_sock = Ssl.embed_socket sock ctx in */
  while (true) {
    log("listening for connections");
    let (clientSocket, caller) = Unix.accept(listeningSocket);
    let ssl_s = Ssl.embed_socket(clientSocket, ctx);
    Ssl.accept(ssl_s);
    ignore(Thread.create(handle_connexion, (ssl_s, caller)));
  };
};

let () = {
  let bufsize = 1024;
  let buf = Bytes.create(bufsize);
  Ssl_threads.init();
  Ssl.init();
  establish_threaded_server(
    (addr, ssl) => {
      log("accepted a new connection");
      let loop = ref(true);
      while (loop^) {
        let l = Ssl.read(ssl, buf, 0, bufsize);
        let m = Bytes.sub(buf, 0, l);
        let msg = Bytes.sub(m, 0, Bytes.length(m) - 1);
        log(Printf.sprintf(">> recieved '%S'", msg));
        if (msg == "exit") {
          log("A client has quit");
          Ssl.shutdown(ssl);
          loop := false;
        } else {
          Ssl.output_string(ssl, "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Type: text/plain\r\n\r\nHello");
        };
      };
    },
    Unix.ADDR_INET(Unix.inet_addr_any, port),
    100,
  );
};
