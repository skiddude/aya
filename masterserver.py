#!/usr/bin/env python
"""
Aya Masterserver
A basic masterserver implementing authorization. Feel free to modify as needed.
Requires flask and waitress (`pip install flask waitress`)

./masterserver.py --port 53600 --key <your_authorization_key> (optional)
"""

import json
import time
import uuid
import argparse
import logging
from flask import Flask, request, jsonify
from datetime import datetime
from waitress import serve

class AyaMasterserver:
    def __init__(self, port=53600, authorization_key=None):
        self.app = Flask(__name__)
        self.port = port
        self.authorization_key = authorization_key
        self.servers = {}
        self.setup_routes()
        
        # Configure custom logging format
        logging.basicConfig(
            level=logging.INFO,
            format='[Aya Masterserver] %(message)s'
        )
        self.logger = logging.getLogger(__name__)

    def check_auth(self):
        if not self.authorization_key:
            return True
        auth_header = request.headers.get('Authorization')
        return auth_header == self.authorization_key

    def setup_routes(self):
        @self.app.route('/broadcast', methods=['POST'])
        def broadcast_server():
            if not self.check_auth():
                return jsonify({'error': 'Unauthorized'}), 401

            if not request.is_json:
                return jsonify({'error': 'Content-Type must be application/json'}), 400

            data = request.get_json()
            required_fields = ['server_host', 'server_name', 'server_port', 'server_ip', 'players', 'max_players']
            
            for field in required_fields:
                if field not in data:
                    return jsonify({'error': f'Missing required field: {field}'}), 400

            try:
                server_port = int(data['server_port'])
                players = int(data['players'])
                max_players = int(data['max_players'])
            except (ValueError, TypeError):
                return jsonify({'error': 'server_port, players, and max_players must be integers'}), 400

            request_ip = request.remote_addr
            if request.headers.get('X-Forwarded-For'):
                request_ip = request.headers.get('X-Forwarded-For').split(',')[0].strip()

            server_id = str(uuid.uuid4())
            server_info = {
                'server_id': server_id,
                'server_host': data['server_host'],
                'server_name': str(data['server_name']),
                'server_port': server_port,
                'server_ip': request_ip,
                'players': players,
                'max_players': max_players,
                'last_ping': time.time()
            }

            self.servers[server_id] = server_info
            self.logger.info(f"New server broadcasting: {server_id} - {data['server_name']}")

            return jsonify({'status': 'success', 'server_id': server_id}), 200

        @self.app.route('/ping', methods=['GET', 'POST'])
        def ping_server():
            if not self.check_auth():
                return jsonify({'error': 'Unauthorized'}), 401

            request_ip = request.remote_addr
            if request.headers.get('X-Forwarded-For'):
                request_ip = request.headers.get('X-Forwarded-For').split(',')[0].strip()

            server_id = request.args.get('id')

            if not server_id:
                return jsonify({'error': 'Missing required field: id'}), 400

            # Check if the server exists
            server_info = self.servers.get(server_id)
            if not server_info:
                return jsonify({'error': 'Server not found'}), 404

            # Verify the IP matches the client_ip for the server
            if server_info['server_ip'] != request_ip:
                return jsonify({'error': 'IP address mismatch for ping - please ping from the IP you broadcasted from'}), 403

            self.servers[server_id]['last_ping'] = time.time()
            return jsonify({'status': 'success'}), 200

        @self.app.route('/', methods=['GET'])
        def get_servers():
            if not self.check_auth():
                return jsonify({'error': 'Unauthorized'}), 401

            # Clean up expired servers (older than 5 minutes)
            current_time = time.time()
            expired = [sid for sid, info in self.servers.items() 
                      if current_time - info['last_ping'] > 300]
            
            for server_id in expired:
                del self.servers[server_id]
                self.logger.info(f"Removed expired server: {server_id}")

            return jsonify({
                'servers': list(self.servers.values()),
                'timestamp': datetime.now().isoformat()
            }), 200

        @self.app.route('/health', methods=['GET'])
        def health_check():
            if not self.check_auth():
                return jsonify({'error': 'Unauthorized'}), 401

            return jsonify({
                'status': 'healthy',
                'timestamp': datetime.now().isoformat(),
                'server_count': len(self.servers)
            }), 200

    def run(self):
        self.logger.info(f"Starting Aya Masterserver on port {self.port}...")
        self.logger.info("Authorization required" if self.authorization_key else "No authorization required")
        serve(self.app, host='0.0.0.0', port=self.port, threads=4)
        print("[Aya Masterserver] Shutting down...")

def main():
    parser = argparse.ArgumentParser(description='Aya Masterserver')
    parser.add_argument('--port', type=int, default=53600, help='Port (default: 53600)')
    parser.add_argument('--key', type=str, default=None, help='Authorization key (default: none)')

    args = parser.parse_args()
    masterserver = AyaMasterserver(port=args.port, authorization_key=args.key)

    try:
        masterserver.run()
    except Exception as e:
        print(f"[Aya Masterserver] Error: {e}")

if __name__ == '__main__':
    main()