<script>
  import { onMount } from "svelte"
  import Card from "@/Components/Card.svelte"
  import Button from "@/Controls/Button.svelte";
  import InputError from "@/Controls/InputError.svelte"

  let transport
  let servers = $state([]);
  let error = $state("");

    onMount(() => {
      new QWebChannel(qt.webChannelTransport, (channel) => {
        transport = channel.objects.jsHelpers
      })
    })

</script>

<Card class="col-span-1 h-full" icon="fa-server">

  {#if error}
    <InputError message={error} />
  {/if}
  
  {#each servers as server, i}
    <div class="flex w-full items-center rounded-md border border-gray-200 bg-gray-100 px-3 py-1.5">
      <div class="w-full">
        <span class="text-gray-600">({server.player_count}/{server.player_limit})</span>
        <span class="font-semibold">{server.host}</span>'s {server.server_name} {i}
	<Button text="Join" class="ms-1 aya-btn-sm" onclick={() => joinServer(i)} />
      </div>
      <!-- <img class="ms-1" height="10" src="./img/pbs.png" /> -->
      <!-- <img class="ms-1" height="10" src="./img/mega.png" /> --> 
    </div>
  {/each}
</Card>
