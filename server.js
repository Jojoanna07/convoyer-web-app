const express = require('express');
const { Server } = require('ws');
const path = require('path');

const PORT = process.env.PORT || 3000;
const app = express();

// Menyajikan file web (index.html) dari folder 'public'
app.use(express.static(path.join(__dirname, 'public')));

// Menjalankan server HTTP
const server = app.listen(PORT, () => {
    console.log(`Server Konveyor berjalan di port ${PORT}`);
});

// Menjalankan server WebSocket (Mak Comblang)
const wss = new Server({ server });

wss.on('connection', (ws) => {
    console.log('Ada perangkat baru yang terhubung!');

    // Kalau server menerima pesan dari Web atau dari ESP32
    ws.on('message', (message) => {
        console.log(`Dapat data: ${message}`);
        
        // Teruskan/Broadcast pesan itu ke SEMUA perangkat yang terhubung (kecuali pengirimnya)
        wss.clients.forEach((client) => {
            if (client !== ws && client.readyState === ws.OPEN) {
                client.send(message.toString());
            }
        });
    });

    ws.on('close', () => {
        console.log('Perangkat terputus.');
    });
});