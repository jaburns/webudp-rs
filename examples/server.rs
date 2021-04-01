use webudp_rs::{WuEvent, WuHost, WuServeTimeout};

fn main() {
    let mut host = WuHost::create("127.0.0.1", "9555", 256);

    println!("WebUDP server listening on localhost:9555...");

    loop {
        while let Some(evt) = host.serve(WuServeTimeout::Immediate) {
            match evt {
                WuEvent::ClientJoin(client) => {
                    println!("Client join: {}", client);
                }
                WuEvent::ClientLeave(client) => {
                    println!("Client leave: {}", client);
                }
                WuEvent::TextData(client, data) => {
                    println!("Receive text: {}", data);
                    host.send_text(client, data.as_str());
                }
                WuEvent::BinaryData(client, data) => {
                    println!("Receive binary: {:?}", data);
                    host.send_binary(client, &data);
                }
            }
        }
        std::thread::sleep_ms(10);
    }
}
