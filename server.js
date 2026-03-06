const express = require('express');
const { Server } = require('ws');
const path = require('path');

const PORT = process.env.PORT || 3000;
const app = express();

// Menampilkan folder 'public' (tempat file index.html berada)
app.use(express.static(path.join(__dirname, 'public')));

const server = app.listen(PORT, () => {
    console.log(`Server Konveyor hidup di port ${PORT}`);
});

// Menjalankan WebSocket Server
const wss = new Server({ server });

wss.on('connection', (ws) => {
    console.log('Ada perangkat yang terhubung ke Railway!');

    ws.on('message', (message) => {
        const data = message.toString();
        console.log(`Terima perintah: ${data}`);
        
        // Teruskan perintah ke semua perangkat lain yang terhubung (termasuk ESP32)
        wss.clients.forEach((client) => {
            if (client !== ws && client.readyState === ws.OPEN) {
                client.send(data);
            }
        });
    });

    ws.on('close', () => console.log('Perangkat terputus.'));
});