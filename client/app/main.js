import "./css/app.css"
import App from "./src/App.svelte"
import { mount } from "svelte";

const params = new URLSearchParams(window.location.search)
const mode = params.get("mode") || "player" // can be of [player, studio, server] to access 3 different UIs

const app = mount(App, {
    target: document.getElementById("app"),
    props: { mode }
})

export default app
