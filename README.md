# libgp
GP is a protocol used to communicate between grumpyd and grumpy

It's a low level TCP protocol that take use of its own serialization mechanism that makes it possible to serialize any Qt type and overrides of libirc::SerializableItem and tranfer it over the network.

Grumpy and grumpyd use this protocol to exchange information. You can use it to connect to them or develop your own application using this technology.
