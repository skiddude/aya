<script>
    let { items = [], activeItem = $bindable(items[0]?.id), onitemchange = null } = $props();

    function handleItemChange(id) {
        activeItem = id
        if (onitemchange) onitemchange(id)
    }
</script>

<div class="flex flex-col">
    {#each items as item}
        {#if activeItem === item.id}
            <div class="flex items-center">
                <button class="aya-games-genre-list w-full !ms-0"><i class={`fa-solid fa-caret-right me-1 text-red-500`}></i> {item.label}</button>
            </div>
        {:else}
            <button class={`aya-games-genre-list ${!item.icon ? 'ml-[10px]' : ''}`} onclick={() => handleItemChange(item.id)}>
                {#if item.icon}
                    <i class={`fa-light fa-fw me-1 ${item.icon}`}></i>
                {/if}
                {item.label}
            </button>
        {/if}
    {/each}
</div>