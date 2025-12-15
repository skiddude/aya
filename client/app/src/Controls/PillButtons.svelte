<script>
    let { items = [], activeItem = $bindable(items[0]?.id), onitemchange = null } = $props();

    function handleItemChange(id) {
        activeItem = id
        if (onitemchange) onitemchange({ id })
    }
</script>

<div class="flex">
    {#each items as item}
        <button class="aya-anim-pop me-1 rounded-full px-3 py-1 text-sm transition duration-200 {activeItem === item.id ? 'border border-aya-500 bg-white font-medium text-aya-500 hover:bg-black/5 dark:border-aya-400 dark:bg-neutral-800 dark:text-aya-400' : 'text-neutral-600 hover:bg-black/10 dark:text-neutral-400 dark:hover:bg-white/10'}" class:active={activeItem === item.id} onclick={() => handleItemChange(item.id)}>
            <i class={`${activeItem === item.id ? "fa-solid" : "fa-regular"} ${item.icon} fa-fw`}></i>
            {item.label}
        </button>
    {/each}
</div>
