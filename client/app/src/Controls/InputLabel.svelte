<script>
    let { value = null, for: forAttr = null, children, ...restProps } = $props()
</script>

<label class="select-none dark:text-neutral-100" for={forAttr} {...restProps}>
    {#if value}
        <span>{value}</span>
    {:else}
        <span>{@render children?.()}</span>
    {/if}
</label>