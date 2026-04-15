"use client";

export default function LogoutButton({ onLogout }) {
  return (
    <button
      type="button"
      onClick={onLogout}
      className="text-glow-danger fixed bottom-[calc(env(safe-area-inset-bottom)+0.75rem)] left-3 z-30 m-2 min-h-11 rounded-lg border border-danger/60 bg-danger/15 px-3 py-2 font-display text-[11px] font-bold uppercase tracking-[0.3em] text-danger backdrop-blur-sm transition-all duration-150 hover:bg-danger/30 hover:shadow-[0_0_16px_color-mix(in_srgb,var(--color-danger)_45%,transparent)] inset-shadow-sm inset-shadow-danger/20 sm:right-[5vw] sm:bottom-[0.5vh] sm:left-auto sm:m-0 sm:min-h-0 sm:px-4 sm:text-xs"
    >
      Logout
    </button>
  );
}
