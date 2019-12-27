<?php
$o = kcp_create(0x11223344);

$socket = stream_socket_client("udp://127.0.0.1:8090", $errno, $errstr);
if (!$socket) {
  echo "$errstr ($errno)<br />\n";
  die;
}

kcp_setoutput($o, function ($o, $data) use ($socket) {
    echo "output\n";
    return fwrite($socket, $data);
});

echo "send msg\n";

while(true) {

    kcp_update($o);

    var_dump(kcp_send($o, "client msg"));
    kcp_flush($o);

    $data = fread($socket, 8192);
    kcp_input($o, $data);

    $msgLen = kcp_peeksize($o);
    while ($msgLen > 0){
        var_dump($msgLen);
        $msg = kcp_recv($o);
        var_dump($msg);
        $msgLen = kcp_peeksize($o);
    }
    sleep(1);
}

