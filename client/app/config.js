import { defineConfig } from "vite";
import { svelte } from "@sveltejs/vite-plugin-svelte";
import autoprefixer from "autoprefixer";
import tailwindcss from "tailwindcss";
import cssnano from "cssnano";
import defaultTheme from "tailwindcss/defaultTheme";
import forms from "@tailwindcss/forms";
import typography from "@tailwindcss/typography";

import path from "path";

export default defineConfig({
  root: path.resolve(__dirname),
  base: "./",
  css: {
    postcss: {
      plugins: [
        tailwindcss(
          /** @type {import('tailwindcss').Config} */
          {
            content: [path.resolve(__dirname, "src") + "/**/*.{html,js,svelte,ts}"],
            darkMode: ["selector", '[data-mode="dark"]'],
            theme: {
              extend: {
                fontFamily: {
                  sans: ["'Source Sans 3'", ...defaultTheme.fontFamily.sans],
                  mono: ["'Source Code Pro'", ...defaultTheme.fontFamily.mono],
                },
                colors: {
                  aya: {
                    100: "#e7d9fd",
                    200: "#ceb3fb",
                    300: "#b68cf9",
                    400: "#9d66f7",
                    500: "#8540f5",
                    600: "#6a33c4",
                    700: "#502693",
                    800: "#351a62",
                    900: "#1b0d31",
                  },
                },
              },
            },

            variants: {
              extend: {
                backgroundColor: ["active", "focus"],
              },
            },

            plugins: [forms, typography],
          }
        ),
        autoprefixer(),
        cssnano(),
      ],
    },
  },
  resolve: {
    alias: {
      "@": path.resolve(__dirname, "src"),
    },
  },
  build: {
    outDir: path.resolve(__dirname, "../common/content/ui"),
    emptyOutDir: true,
    rollupOptions: {
      output: {
        manualChunks: undefined,
        inlineDynamicImports: true,
      },
    },
  },
  plugins: [svelte()],
});
