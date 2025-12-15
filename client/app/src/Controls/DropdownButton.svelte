<script>
    let { shown = false, onclick = () => {}, children } = $props();
</script>

<button {onclick} class="aya-anim-pop rounded-lg text-neutral-500 transition duration-200 hover:text-neutral-600 {shown ? 'bg-black/10' : 'hover:bg-black/5'}">
    {#if children}{@render children({ shown })}{:else}
        <i class="{shown ? 'fa-solid' : 'fa-regular'} fa-caret-down fa-fw aya-anim-pop"></i>
    {/if}
</button>
