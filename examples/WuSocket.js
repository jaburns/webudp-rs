export class WuSocket {
    constructor() {
        this.channel = null;
        this.onmessage = null;
        this.onopen = null;
        this.onclose = null;
        this.open = false;
    }

    connect(address) {
        const peer = new RTCPeerConnection({ iceServers: [{ urls: ["stun:xxx"] }] });

        this.channel = peer.createDataChannel("webudp", {
            ordered: false,
            maxRetransmits: 0
        });
        this.channel.binaryType = "arraybuffer";

        this.channel.onopen = () => {
            this.open = true;
            if (this.onopen) {
                this.onopen();
            }
        };

        this.channel.onclose = () => {
            this.open = false;
            if (this.onclose) {
                this.onclose();
            }
        };

        this.channel.onerror = e =>
            console.error("Data channel error:", e);

        this.channel.onmessage = e => {
            if (this.onmessage) {
                this.onmessage(e);
            }
        };

        const createOffer = async () => {
            const offer = await peer.createOffer();
            await peer.setLocalDescription(offer);

            const response = await new Promise((resolve, reject) => {
                const request = new XMLHttpRequest();
                request.open("POST", address);
                request.onload = () => {
                    if (request.status === 200) {
                        resolve(JSON.parse(request.responseText));
                    } else {
                        reject();
                    }
                };
                request.send(peer.localDescription.sdp);
            });

            await peer.setRemoteDescription(new RTCSessionDescription(response.answer));
            const candidate = new RTCIceCandidate(response.candidate);
            await peer.addIceCandidate(candidate);
        };

        void createOffer();
    }

    send(data) {
        if (this.open) {
            this.channel.send(data);
        }
    }

    close() {
        this.channel.close();
    }
}
