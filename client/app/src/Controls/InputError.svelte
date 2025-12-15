<script>
    /**
     * @typedef {Object} Props
     * @property {any} [message]
     */

    /** @type {Props} */
    let { message = null } = $props();
</script>

{#if message}
    <p class="mt-2 text-sm text-red-600 dark:text-red-400">
        {message}
    </p>
{/if}
