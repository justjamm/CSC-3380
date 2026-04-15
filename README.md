# CSC-3380 Home Security System

Full-stack home security project with:

- `frontend` (Next.js dashboard)
- `middleware` (C++ API + auth + alert aggregation)
- `backend` (C++ RTSP stream service)
- `mongo` + `mediamtx` supporting services

## Quick Setup (Judges/Graders)

### 1. Prerequisite

- Docker Desktop (or Docker Engine + Docker Compose)

### 2. Configure environment

```bash
cp .env.example .env
```

Edit `.env` and set these values:

- `RTSP_USER`, `RTSP_PASS`
- `RTSP_HOST`, `RTSP_HOST_2`, `RTSP_HOST_3`
- `RTSP_PORT`, `RTSP_PATH`
- `EMAIL_USER`, `EMAIL_PASS` (used to send OTP codes)

Tip: if you only have one camera, set all `RTSP_HOST*` values to the same host.

### 3. Start everything

```bash
docker compose up --build
```

### 4. Open the app

- Frontend: `http://localhost:3000`

### 5. Login (seeded account)

- Email: `admin@example.com`
- Password: `admin123`

You will receive OTP at the configured email destination.

## If OTP Email Is Not Configured

After clicking login once, you can read the OTP directly from Mongo (local grading fallback):

```bash
docker compose exec mongo mongosh edr --quiet --eval 'db.users.find({}, { _id:0, email:1, otpCode:1, otpExpiresAt:1 }).pretty()'
```

Use the `otpCode` shown for `admin@example.com` on the verify page.

## Health Checks

```bash
curl http://localhost:8085/health
curl http://localhost:8082/health
```

## Stop

```bash
docker compose down
```

## More Documentation

- Frontend details: `frontend/README.md`
- Middleware details: `middleware/README.MD`
- Contracts: `contracts/README.md`
- Test plans: `tests/README.md`
