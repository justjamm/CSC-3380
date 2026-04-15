# Duncan Home Security Frontend

This folder contains the Next.js dashboard used for operator authentication, camera monitoring, and alert visibility.

## Current features

- Registration, login, and OTP verification flows.
- Protected dashboard route with local token/session persistence.
- Camera list loading and camera selection.
- Live stream image refresh loop (requests `/stream/:id` repeatedly).
- Motion/security alert overlay fed by polling and realtime events.
- Realtime websocket status and updates (`/ws` via the `/api` proxy).
- Camera map panel/toggle UI.

## Requirements

- Node.js 20+
- npm 10+

## Local development

1. Install dependencies:

   ```bash
   cd frontend
   npm install
   ```

2. (Optional but recommended) Create `frontend/.env.local`:

   ```env
   # Where Next.js forwards /api/* requests during local dev
   STREAM_INTERNAL_URL=http://localhost:8085

   # Used for websocket URL fallback logic outside browser context
   NEXT_PUBLIC_API_URL=http://localhost:8085

   # Next.js dev origin allowlist
   ALLOWED_DEV_ORIGINS=localhost,127.0.0.1
   ```

3. Start the app:

   ```bash
   npm run dev
   ```

4. Open `http://localhost:3000`.

## Scripts

- `npm run dev` - start Next.js dev server.
- `npm run build` - create production build.
- `npm run start` - run production server.
- `npm run lint` - run ESLint.

## API and proxy behavior

- Browser requests are made to same-origin `/api/*`.
- `next.config.mjs` rewrites `/api/:path*` to:
  `STREAM_INTERNAL_URL` -> `NEXT_PUBLIC_API_URL` -> `http://localhost:8085`.
- Websocket connections use `/api/ws?token=...` in browser contexts.

## Docker compose flow

When started from repo root (`docker compose up --build`):

- Frontend is exposed on host port `${FRONTEND_PORT:-3001}`.
- Middleware is expected at `http://middleware:8080` inside the Docker network.

## Key files

- `app/page.js` - main authenticated dashboard.
- `app/lib/apiClient.js` - central API client and websocket URL helpers.
- `app/providers/AuthProvider.js` - auth state + token persistence.
- `app/useMjpegStream.js` - frame polling hook for stream rendering.
- `next.config.mjs` - `/api` rewrite and dev origin configuration.
