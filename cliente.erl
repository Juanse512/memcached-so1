-module(cliente).
-export([start/2, get/2, del/2, stats/1, put/3, test/1]).

generate_bin(3) -> <<0,0,0>>;
generate_bin(2) -> <<0,0>>;
generate_bin(1) -> <<0>>;
generate_bin(N) -> <<>>.

compare_response(V) ->
    AsciiCode = V,
    case AsciiCode of
        111 -> einval;
        112 -> enotfound;
        101 -> ok
    end.


start(Hostname,Port) ->
    {ok, Sock} = gen_tcp:connect(Hostname, Port, 
                                 [binary, {packet, 0}]),
    Sock.

decode(Tail) ->
    [A, B, C, D | Ntail] = Tail,
    Ntail.

get(Sock,Key) ->
    Comm = binary:encode_unsigned(13,big),
    Bin = term_to_binary(Key),
    BinR = binary:part(Bin, {byte_size(Bin), 2-byte_size(Bin)}),
    Rest = generate_bin(4 - (byte_size(BinR)-string:len(Key))),
    R = <<Comm/binary, Rest/binary, BinR/binary>>,
    % io:fwrite("~p ~p ~n", [R, (4 - (byte_size(BinR)-string:len(Key)))]),
    gen_tcp:send(Sock, R),
    receive
        {tcp, Sock, Paquete} -> List = binary_to_list(Paquete),
                                [Head | Tail] = List,
                                Response = compare_response(Head),
                                case Response of
                                    ok -> {ok, decode(Tail)}; 
                                    _X -> _X
                                end
    end.
del(Sock,Key) ->
    Comm = binary:encode_unsigned(12,big),
    Bin = term_to_binary(Key),
    BinR = binary:part(Bin, {byte_size(Bin), 2-byte_size(Bin)}),
    Rest = generate_bin(4 - (byte_size(BinR)-string:len(Key))),
    R = <<Comm/binary, Rest/binary, BinR/binary>>,
    % io:fwrite("~p ~p ~n", [R, (4 - (byte_size(BinR)-string:len(Key)))]),
    gen_tcp:send(Sock, R),
    receive
        {tcp, Sock, Paquete} -> List = binary_to_list(Paquete),
                                [Head | Tail] = List,
                                compare_response(Head)
    end.

stats(Sock) ->
    Comm = binary:encode_unsigned(21,big),
    R = <<Comm/binary>>,
    % io:fwrite("~p ~p ~n", [R, (4 - (byte_size(BinR)-string:len(Key)))]),
    gen_tcp:send(Sock, R),
    receive
        {tcp, Sock, Paquete} -> List = binary_to_list(Paquete),
                                [Head | Tail] = List,
                                Response = compare_response(Head),
                                case Response of
                                    ok -> {ok, decode(Tail)}; 
                                    _X -> _X
                                end
    end.

put(Sock,Key,Value) ->
    Comm = binary:encode_unsigned(11,big),
    BinK = term_to_binary(Key),
    BinRK = binary:part(BinK, {byte_size(BinK), 2-byte_size(BinK)}),
    RestK = generate_bin(4 - (byte_size(BinRK)-string:len(Key))),
    BinV = term_to_binary(Value),
    BinRV = binary:part(BinV, {byte_size(BinV), 2-byte_size(BinV)}),
    RestV = generate_bin(4 - (byte_size(BinRV)-string:len(Value))),
    R = <<Comm/binary, RestK/binary, BinRK/binary, RestV/binary, BinRV/binary>>,
    % io:fwrite("~p ~p ~n", [R, (4 - (byte_size(BinR)-string:len(Key)))]),
    gen_tcp:send(Sock, R),
    receive
        {tcp, Sock, Paquete} -> List = binary_to_list(Paquete),
                                [Head | Tail] = List,
                                compare_response(Head)
    end.


test(N) -> 
    Sock = start("localhost", 889),
    testAux(Sock, N).
testAux(Sock, 0) -> ok;
testAux(Sock, N) -> put(Sock, integer_to_list(N), integer_to_list(N)),
                    testAux(Sock, N-1).