"use client";

export default function LogoutButton({ onLogout }) {
  return (
    <button
      type="button"
      onClick={onLogout}
      className="text-glow-green fixed bottom-[5vh] left-[5vw] z-30 rounded-lg border border-red-700/50 bg-red-900/50 px-4 py-2 font-mono text-xs uppercase tracking-wider text-red-300 backdrop-blur-sm transition-colors hover:bg-red-800/70"
    >
      Logout
    </button>
  );
}
