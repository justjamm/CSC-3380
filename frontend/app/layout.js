import { Orbitron, JetBrains_Mono } from "next/font/google";
import "./globals.css";
import { AuthProvider } from "./providers/AuthProvider";
import ThemeShell from "./components/ThemeShell";

const displayFont = Orbitron({
  subsets: ["latin"],
  weight: ["400", "600", "700", "900"],
  variable: "--font-display-src",
  display: "swap",
});

const monoFont = JetBrains_Mono({
  subsets: ["latin"],
  weight: ["400", "500", "700"],
  variable: "--font-mono-src",
  display: "swap",
});

export const metadata = {
  title: "IoT EDR Hub",
  description: "IoT Endpoint Detection and Response Dashboard",
};

const PALETTE_BOOTSTRAP = `
(function(){try{var p=localStorage.getItem('hud.palette');if(!p||!['corpo-green','militech-red','arasaka-amber','neon-yellow','night-city'].includes(p)){p='militech-red';}document.documentElement.dataset.palette=p;}catch(e){document.documentElement.dataset.palette='militech-red';}})();
`;

export default function RootLayout({ children }) {
  return (
    <html
      lang="en"
      className={`dark ${displayFont.variable} ${monoFont.variable}`}
      data-palette="militech-red"
    >
      <head>
        <script dangerouslySetInnerHTML={{ __html: PALETTE_BOOTSTRAP }} />
      </head>
      <body>
        <AuthProvider>
          <ThemeShell>{children}</ThemeShell>
        </AuthProvider>
      </body>
    </html>
  );
}
