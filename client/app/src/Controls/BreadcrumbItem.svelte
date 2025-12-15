<script>
    /**
     * @typedef {Object} Props
     * @property {boolean} [active]
     * @property {import('svelte').Snippet} [children]
     */

    /** @type {Props} */
    let { active = false, children } = $props();
</script>

<div class="dark:text-white {active ? '' : 'me-2'}">
    {@render children?.()}

    {#if !active}
        <i class="fa-light fa-chevron-right fa-fw text-neutral-500"></i>
    {/if}
</div>
