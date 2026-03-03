import "./globals.css";

export const metadata = {
  title: "IoT EDR Hub",
  description: "IoT Endpoint Detection and Response Dashboard",
};

export default function RootLayout({ children }) {
  return (
    <html lang="en" className="dark">
      <body>{children}</body>
    </html>
  );
}
