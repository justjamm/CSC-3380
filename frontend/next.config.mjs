/** @type {import('next').NextConfig} */
const streamInternalUrl = (process.env.STREAM_INTERNAL_URL || "http://localhost:8082").replace(/\/+$/, "");
const allowedDevOrigins = (
  process.env.ALLOWED_DEV_ORIGINS ||
  "localhost,127.0.0.1,192.168.0.213"
)
  .split(",")
  .map((origin) => origin.trim())
  .filter(Boolean);

const nextConfig = {
  allowedDevOrigins,
  async rewrites() {
    return [
      {
        source: "/api/stream",
        destination: `${streamInternalUrl}/stream`,
      },
      {
        source: "/api/stream/:path*",
        destination: `${streamInternalUrl}/stream/:path*`,
      },
      {
        source: "/api/mjpeg",
        destination: `${streamInternalUrl}/mjpeg`,
      },
      {
        source: "/api/mjpeg/:path*",
        destination: `${streamInternalUrl}/mjpeg/:path*`,
      },
      {
        source: "/api/stream-health",
        destination: `${streamInternalUrl}/health`,
      },
    ];
  },
};

export default nextConfig;
