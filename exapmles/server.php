<?php

$socket = stream_socket_server("udp://0.0.0.0:8090", $errno, $errstr, STREAM_SERVER_BIND);
if (!$socket) {
  echo "$errstr ($errno)<br />\n";
  die;
}

class Session
{
    private $kcp;
    private $socket;
    private $address;

    function __construct($socket, $address)
    {
        $this->kcp = kcp_create(0x11223344);
        $this->socket = $socket;
        $this->address = $address;
        kcp_setoutput($this->kcp, [$this, 'send']);
    }

    function send()
    {

    }

    function input($data)
    {
        $o = $this->kcp;
        kcp_update($o);
        kcp_input($o, $data);

        echo "peek\n";
        $msgLen = kcp_peeksize($o);
        while ($msgLen > 0){
            var_dump($msgLen);
            $msg = kcp_recv($o, $msgLen);
            var_dump($msg);
    
            $n = kcp_send($o, "hello world");
            var_dump($n);
            kcp_flush($o);
            $msgLen = kcp_peeksize($o);
    
            sleep(1);    
            echo "msg\n";
        }
    }

    function output()
    {

    }
}

$session_list = [];

while(true) {
  
    $data = stream_socket_recvfrom($socket, 8192, 0, $address);
    if (strlen($data) <= 0) {
        continue;
    }
    if (!array_key_exists($address, $session_list)) {
        $session_list[$address] = new Session($socket, $address);
    }

    $session = $session_list[$address];
    $session->input($data);


    sleep(1);
}

