# QSO Globe Notification Protocol

Protocol version: QSO1

## Transport

UDP over IPv4.

Default destination:

    127.0.0.1:45832

The receiver binds exclusively to the IPv4 loopback interface.

The listening port is configurable.

## Datagram format

Each UDP datagram contains exactly four tab-separated fields:

    QSO1<TAB>ID<TAB>CALLSIGN<TAB>LOCATOR

No trailing newline or null terminator is required or permitted
as part of the locator field.

## Fields

### Protocol identifier

The literal ASCII string QSO1.

### Contact identifier

A nonzero unsigned 64-bit decimal integer.

Permitted range:

    1 through 18446744073709551615

Identifiers should be unique within the notification stream.

### Callsign

Between 3 and 20 characters.

Permitted characters:

- Uppercase ASCII letters A-Z.
- Digits 0-9.
- Forward slash.
- Hyphen.

### Locator

A valid four-, six-, or eight-character Maidenhead locator.

Locator letters are accepted in either uppercase or lowercase.

The geographic center of the locator cell is used as the
destination coordinate.

## Example

    QSO1<TAB>900001<TAB>KD8WCK<TAB>EN91JF

## Perl notification example

    use IO::Socket::INET;

    my $socket = IO::Socket::INET->new(
        PeerAddr => "127.0.0.1",
        PeerPort => 45832,
        Proto    => "udp"
    ) or die $!;

    $socket->send(
        join("\t", "QSO1", 900001,
             "KD8WCK", "EN91JF")
    ) or die $!;

## Receiver behavior

Valid notifications are placed into a FIFO queue.

The queue accommodates 128 pending contacts.

A bounded duplicate history retains 1024 accepted identifiers.

Malformed datagrams are rejected without modifying the
previously decoded event.

The receiver rejects zero-length datagrams, datagrams of
256 bytes or more, and datagrams containing embedded nulls.

When the pending queue is full, new contacts are rejected.

## Delivery guarantees

The protocol is connectionless and does not implement
acknowledgments, retransmission, authentication, or persistent
delivery.

A successful UDP send does not guarantee that a notification
was received or processed.

Duplicate suppression is confined to the lifetime of the
running application.

## Integration policy

Logging software remains authoritative for contact records.

QSO Globe is a visualization application and does not modify
the logging database.

Notification delivery should not prevent a logging application
from completing its normal contact-recording operation.
