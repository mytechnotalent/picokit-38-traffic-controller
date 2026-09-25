# Protocol

## Transport

The node and the gateway each carry an RYLR998 LoRa module. The node uses
UART1 (GP8/GP9); the gateway uses the second module on the computer USB serial
port. The modules are configured with AT commands:

```
AT+ADDRESS=<node>
AT+NETWORKID=<net>
AT+BAND=<hz>
AT+PARAMETER=<sf>,<bw>,<cr>,<preamble>
AT+SEND=<dest>,<len>,<payload>
+RCV=<src>,<len>,<data>,<rssi>,<snr>
```

## Payload

The payload is the lowercase hex encoding of a sealed envelope. The plaintext
telemetry is JSON:

```
{"n":38,"s":12,"p":2}
```

where `n` is the node id, `s` is the monotonic sequence number, and `p` is the
traffic phase (`0` red, `1` green, `2` yellow, `3` pedestrian).

## Envelope

```
nonce(24) | ciphertext | tag(16)
```

The nonce is random per frame. The associated data is the single byte node id.
The envelope is hex encoded for the AT payload path.

## Key

The field key is 32 bytes derived with Argon2id:

- passphrase: `picokit field key v1`
- salt: `picokit-salt-0001` (16 bytes)
- time cost 3, parallelism 1, memory 64 blocks

In production the key is provisioned per device through OTP; the passphrase
here is a lab default.

## Phases

The controller cycles red (3 s), green (4 s), and yellow (2 s). A latched
pedestrian request replaces the yellow-to-red transition with a 5 s pedestrian
phase that holds the red lamp.
