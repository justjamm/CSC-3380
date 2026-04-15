/** @type {import('next').NextConfig} */
const middlewareInternalUrl = (
  process.env.STREAM_INTERNAL_URL ||
  process.env.NEXT_PUBLIC_API_URL ||
  "http://localhost:8085"
).replace(/\/+$/, "");
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
        source: "/api/:path*",
        destination: `${middlewareInternalUrl}/:path*`,
      },
    ];
  },
};

export default nextConfig;
