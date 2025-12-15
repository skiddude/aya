<script>
    let { page, selectedPage, onClick } = $props();
</script>

<button onclick={onClick} data-page={page.id} title={page.title} class="mb-1.5 rounded-lg p-2 {page.id == selectedPage ? 'aya-page-btn-selected' : 'aya-page-btn'}">
    {#if page.id == "about" || page.id == "settings"}
        <i class="{page.id == selectedPage ? 'fa-solid' : 'fa-light'} {page.icon} fa-fw fa-lg text-zinc-800 dark:text-neutral-50"></i>
    {:else}
        <img src="./img/icons/{page.id}.png" alt={page.title} />
    {/if}
</button>
