<?php
$serv1 = new HttpServer(82, "tcp://0.0.0.0");
$serv1->setPublicPath(__DIR__);

$serv1->on_connect(function() {
    echo"request started\n";
});

$serv1->on_disconnect(function() {
    echo"request ended\n";
});
file_get_contents_async('./stubs/serv.php', function($arg) use ($res){
    echo "file is read before\n";}

//      $res->setHeader("Content-Type", "text/plain; charset=utf-8");
//        $res->send("hello\n");
//     }
    );
$serv1->on_request(function (HttpRequest $req, HttpResponse $res) use ($serv1) {
//     file_get_contents_async('./stubs/index.php', function($arg) use ($res){
//     echo "file is read\n";
//      $res->setHeader("Content-Type", "text/plain; charset=utf-8");
//        $res->send("hello\n");
//     }
//     );
set_timeout(function() use ($res){
//  $res->setHeader("Content-Type", "text/plain; charset=utf-8");
//        $res->send("hello\n");
echo "TIMEOUT";
}, 2000);
    echo "why\n\n";
         $res->setHeader("Content-Type", "text/plain; charset=utf-8");
            $res->send("hello\n");
//    var_dump($req, $res, $this);
    if (file_exists($serv1->publicPath.$req->uri) && !is_dir($serv1->publicPath.$req->uri)){

        echo  "WTF???";

        $result = new finfo();
         $mimeType = $result->file($serv1->publicPath.$req->uri, FILEINFO_MIME_TYPE);

        $file = file_get_contents($serv1->publicPath.$req->uri);
        echo "1234 $serv1->publicPath.$req->uri\n";

        echo  $mimeType;
       // gc_collect_cycles(); //triggers error
       $res->setHeader("Content-Type", "mimeType")
            ->setHeader("Content-Length", strlen($file));
       $res->send($file);

       return;
    }



});
