# Aya Masterserver

- Default port: 53600
- Run command: `./masterserver.py --port 53600 --key <optional_auth_key>`
- Communicates using `application/json`
- Requires an `Authorization ` header if a key is provided
- Missing or invalid key returns `401 Unauthorized`
- Invalid request returns `400 Bad Request`
- Responses are always JSON: `{"status": "success" or "error", ...}` (includes "error" field including message if except. occurs.)

## POST `/broadcast`

- Registers a new server
- Required fields:
  - `server_host` (str): Hostname of the server
  - `server_name` (str): Display name of the server
  - `server_port` (int): Port number
  - `server_ip` (str): Public IP address
  - `players` (int): Current number of players
  - `max_players` (int): Maximum number of players
- Response: `{"status": "success", "server_id": "<unique_id>"}`
- Save the `server_id` for future pings

## POST/GET `/ping`

- Updates or verifies a server’s current status
- Checks that the provided IP matches the registered one
- Required fields:
  - `server_id` (str): Unique ID from /broadcast
  - `server_name` (str): Server name
  - `server_port` (int): Server port
  - `server_ip` (str): Server IP
  - `server_host` (str): Server hostname
  - `players` (int): Current players
  - `max_players` (int): Maximum players

## GET `/health`

- Returns basic health information
- Example response: `{"timestamp": "<ISO 8601>", "server_count": <int>}`

## GET `/`

- Returns all currently registered servers
- Example response:

```json
{
    "servers": [
        {
            "id": "<string>",
            "host": "<string>",
            "name": "<string>",
            "port": <int>,
            "ip": "<string>",
            "players": <int>,
            "max_players": <int>
        }
    ],
    "count": <int>
}
```
