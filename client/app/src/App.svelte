<script>
    import { onMount } from "svelte"
    import PlayerApp from "@/Player.svelte"
    import StudioApp from "@/Studio.svelte"
    import ServerApp from "@/Server.svelte"
    
    let { mode } = $props();

    let transport = $state(null)

    onMount(() => {
        new QWebChannel(qt.webChannelTransport, (channel) => {
            transport = channel.objects.transport
        })
    })
</script>

{#if mode === "player"}
    <PlayerApp {transport} />
{:else if mode === "studio"}
    <StudioApp {transport} />
{:else if mode === "server"}
    <ServerApp {transport} />
{/if}
