# Duncan Home Security Middleware

This directory contains the backend integration layer used by the frontend dashboard. It includes multiple services that work together for auth, camera stream routing, and alert aggregation.

## Services in this folder

- `middleware` (C++, `src/main.cpp`): main API service.
- `auth-service` (Node.js): register/login/OTP verification and JWT issuing.
- `motion-detection` (C++/OpenCV): motion analysis and alert feed.
- `api-gateway` (Node.js): legacy placeholder proxy (not used by current `docker-compose.yml`).

## Recommended startup (repo root)

From `/data/CSC-3380`:

```bash
docker compose up --build
```

Default host ports from compose:

- Frontend: `3001`
- Middleware API: `8085` (container listens on `8080`)
- Backend: `8082`
- MongoDB: `27018`

## Main middleware API surface

Public routes exposed by `middleware/src/App.cpp`:

- `GET /health`
- `GET /health/live`
- `GET /health/ready`
- `POST /auth/register` (proxied to `auth-service`)
- `POST /auth/login` (proxied to `auth-service`)
- `POST /auth/verify-otp` (proxied to `auth-service`)
- `POST /camera/select` (requires `Authorization: Bearer <token>`)
- `GET /devices` (requires auth; reads camera IDs from backend `/cameras`)
- `GET /stream/:id` (requires auth; single-frame proxy)
- `GET /mjpeg/:id` (requires auth; chunked MJPEG stream)
- `GET /alerts` and `GET /edr/alerts` (requires auth; aggregated alerts)
- `GET /ws?token=<jwt>` websocket for realtime `device_update`, `alert`, and `heartbeat` events.

## Environment variables

### C++ middleware (`middleware/.env`)

Template: `config/.env.example`

- `HOST` (default `0.0.0.0`)
- `PORT` (default `8080`)
- `BACKEND_HOST` (required)
- `BACKEND_PORT` (required)
- `STREAM_HOST` (optional, defaults to `BACKEND_HOST`)
- `STREAM_PORT` (optional, defaults to `BACKEND_PORT`)
- `JWT_SECRET` (required)
- `LOG_LEVEL` (`debug|info|warn|error`, default `info`)
- `EDR_POLL_MS` (default `500`)

### `auth-service`

- `PORT` (default `3002`)
- `MONGO_URI` (default `mongodb://mongo:27017/edr`)
- `JWT_SECRET`
- `JWT_ISSUER` (default `middleware`)
- `JWT_AUDIENCE` (default `frontend`)
- `OTP_EXPIRY_SECS` (default `300`)
- `EMAIL_USER` and `EMAIL_PASS` (required for sending OTP emails)

### `motion-detection`

- `BACKEND_URL` (default `http://backend:8080`)
- `CAMERAS` (default `cam1,cam2,cam3`)
- `IDLE_DROP` (default `5`)
- `ACTIVE_DROP` (default `1`)
- `COOLDOWN_FRAMES` (default `30`)

## Local native build (C++ middleware only)

```bash
cd middleware
cp config/.env.example .env
cmake -S . -B build
cmake --build build -j"$(nproc)"
./build/middleware
```

### Native build requirements

- CMake 3.16+
- C++23 compiler (project currently uses GCC in Docker)
- OpenSSL development libraries
- pthreads

`nlohmann_json` and `jwt-cpp` are fetched by CMake when unavailable locally.

## Local auth-service run

```bash
cd middleware/auth-service
npm install
npm start
```

## Local motion-detection run

```bash
cd middleware/motion-detection
mkdir -p build
cd build
cmake ..
make -j"$(nproc)"
./src/motion_detection
```

Requires OpenCV and libcurl development packages.

## Important auth compatibility note

The middleware JWT validator in `src/App.cpp` is currently initialized with a hardcoded secret (`middleware-dev-signing-secret`). Make sure `auth-service` uses the same JWT secret value, or bearer-token validation will fail for protected routes.

## Dependency note (`httplib.h`)

`include/httplib.h` is already present in this repository.

If you ever need to refresh it manually:

```bash
cd middleware/include
curl -o httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
```
