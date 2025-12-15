<script>

    import IDEPage from "@/Pages/Studio/IDE.svelte"
    import ToolboxPage from "@/Pages/Studio/Toolbox.svelte"
    let { transport } = $props();

    const params = new URLSearchParams(window.location.search)
    const page = params.get("page") || "ide" // can be of [ide, toolbox] to access two different UIs
</script>

{#if page === "ide"}
    <IDEPage {transport} />
{:else if page === "toolbox"}
    <ToolboxPage {transport} />
{/if}
