<script>
    let { items = [], activeItem = $bindable(items[0]?.id), onitemchange = null } = $props();

    function handleItemChange(id) {
        activeItem = id
        if (onitemchange) onitemchange(id)
    }
</script>

{#each items as item}
    <button class="aya-anim-pop flex flex-nowrap items-center rounded-lg px-2 py-1 text-start transition duration-200 {activeItem === item.id ? 'bg-black/10 dark:bg-white/10 font-semibold text-aya-700 dark:text-aya-500' : 'text-aya-600/85 hover:bg-black/5 dark:hover:bg-white/5 hover:text-aya-600 dark:text-aya-600'}" class:active={activeItem === item.id} onclick={() => handleItemChange(item.id)}>
        <i class={`${activeItem === item.id ? "fa-solid" : "fa-regular"} ${item.icon} fa-fw me-1`}></i>
        {item.label}
    </button>
{/each}
