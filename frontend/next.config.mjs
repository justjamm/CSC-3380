/** @type {import('next').NextConfig} */
const streamInternalUrl = (process.env.STREAM_INTERNAL_URL || "http://localhost:8082").replace(/\/+$/, "");

const nextConfig = {
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
