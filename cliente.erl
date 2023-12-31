-module(cliente).
-export([start/2, get/2, del/2, stats/1, put/3, test/1, test2/1, closeSocket/1, test3/1, test4/1]).

% generate_bin: Genera un binario con una cantidad N de ceros.
generate_bin(3) -> <<0,0,0>>;
generate_bin(2) -> <<0,0>>;
generate_bin(1) -> <<0>>;
generate_bin(_) -> <<>>.

% compare_response: Dado un codigo devuelve el átomo correspondiente a esa respuesta
compare_response(V) ->
    AsciiCode = V,
    case AsciiCode of
        111 -> einval;
        112 -> enotfound;
        115 -> eunk;
        116 -> emem;
        101 -> ok;
        X -> X
    end.

% start: Dado el host y el puerto devuelve el socket conectado al servidor
start(Hostname,Port) ->
    {ok, Sock} = gen_tcp:connect(Hostname, Port, 
                                 [binary, {packet, 0}]),
    Sock.
 

decode([]) -> vacio;
decode(Tail) ->
    [_, _, _, _ | Ntail] = Tail,
    Ntail.

% get: Dado un socket y una key genero una instruccion GET con esa key
get(Sock,Key) ->
    Comm = binary:encode_unsigned(13,big),
    Bin = term_to_binary(Key),
    BinR = binary:part(Bin, {byte_size(Bin), 2-byte_size(Bin)}),
    Rest = generate_bin(4 - (byte_size(BinR)-string:len(Key))),
    R = <<Comm/binary, Rest/binary, BinR/binary>>,
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

% del: Dado un socket y una key genero una instruccion DEL con esa key
del(Sock,Key) ->
    Comm = binary:encode_unsigned(12,big),
    Bin = term_to_binary(Key),
    BinR = binary:part(Bin, {byte_size(Bin), 2-byte_size(Bin)}),
    Rest = generate_bin(4 - (byte_size(BinR)-string:len(Key))),
    R = <<Comm/binary, Rest/binary, BinR/binary>>,
    gen_tcp:send(Sock, R),
    receive
        {tcp, Sock, Paquete} -> List = binary_to_list(Paquete),
                                [Head | _] = List,
                                compare_response(Head)
    end.
% stats: Dado un socket genero una instruccion STATS
stats(Sock) ->
    Comm = binary:encode_unsigned(21,big),
    R = <<Comm/binary>>,
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

% put: Dado un socket, una key y un value genera una instruccion PUT con esa key asociado al value
put(Sock,Key,Value) ->
    Comm = binary:encode_unsigned(11,big),
    BinK = term_to_binary(Key),
    BinRK = binary:part(BinK, {byte_size(BinK), 2-byte_size(BinK)}),
    RestK = generate_bin(4 - (byte_size(BinRK)-string:len(Key))),
    BinV = term_to_binary(Value),
    BinRV = binary:part(BinV, {byte_size(BinV), 2-byte_size(BinV)}),
    RestV = generate_bin(4 - (byte_size(BinRV)-string:len(Value))),
    R = <<Comm/binary, RestK/binary, BinRK/binary, RestV/binary, BinRV/binary>>,
    gen_tcp:send(Sock, R),
    receive
        {tcp, Sock, Paquete} -> List = binary_to_list(Paquete),
                                [Head | _] = List,
                                compare_response(Head)
        after 1 -> timeout
    end.

test(N) -> 
    Sock = start("localhost", 889),
    testAux(Sock, N).
testAux(_, 0) -> [];
testAux(Sock, N) -> R = put(Sock, integer_to_list(N), integer_to_list(N)),
                    [R] ++ testAux(Sock, N-1).

test2(N) -> 
    Sock = start("localhost", 889),
    testAux2(Sock, N).
testAux2(_, 0) -> [];
testAux2(Sock, N) -> R = del(Sock, integer_to_list(N)),
                     [R] ++ testAux2(Sock, N-1).

test3(N) -> 
    Sock = start("localhost", 889),
    testAux3(Sock, N).
testAux3(_, 0) -> [];
testAux3(Sock, N) -> R1 = put(Sock, integer_to_list(N), integer_to_list(N)),
                    %  R2 = get(Sock, integer_to_list(N+1)),
                     R3 = del(Sock, integer_to_list((N+1))),
                     [R1] ++ [R3] ++ testAux3(Sock, N-1).

test4(N) -> 
    Sock = start("localhost", 889),
    testAux4(Sock, N).
testAux4(_, 0) -> [];
testAux4(Sock, N) -> R1 = get(Sock, integer_to_list(N)),
                     [R1] ++ testAux4(Sock, N-1).


closeSocket(Sock) -> gen_tcp:shutdown(Sock, read_write).